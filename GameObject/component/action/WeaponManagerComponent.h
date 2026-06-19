#pragma once
#include <memory>
#include <vector>
#include "engine/gameobject/component/base/IActionComponent.h"
#include "application/GameObject/component/action/IWeaponComponent.h"

class Object3dCommon;
class LightManager;

namespace GameObjectComponent
{
    /**
     * @brief 武器管理コンポーネント
     *
     * プレイヤーの武器を管理し、切り替えとUpdateを一元的に行う。
     * 武器はこのコンポーネントが所有し、選択中の武器のみがプレイヤー入力を処理する。
     */
    class WeaponManagerComponent : public IActionComponent
    {
    public:
        /**
         * @brief コンストラクタ
         * @param object3dCommon 3Dオブジェクト共通情報
         * @param lightManager ライトマネージャー
         */
        WeaponManagerComponent(Object3dCommon* object3dCommon, LightManager* lightManager);

        /**
         * @brief デストラクタ
         */
        ~WeaponManagerComponent() override = default;

        /**
         * @brief 武器を追加
         * @param weapon 追加する武器（所有権を移譲）
         */
        void AddWeapon(std::unique_ptr<IWeaponComponent> weapon);

        /**
         * @brief 毎フレームの更新処理
         * @param owner このコンポーネントを所有するゲームオブジェクト
         */
        void Update(::GameObject* owner) override;

        /**
         * @brief 描画処理
         * @param camera カメラマネージャー
         */
        void Draw3D(CameraManager* camera) override;

        /**
         * @brief 次の武器に切り替え
         */
        void SwitchToNext();

        /**
         * @brief 前の武器に切り替え
         */
        void SwitchToPrevious();

        /**
         * @brief 指定インデックスの武器に切り替え
         * @param index 武器のインデックス（0始まり）
         */
        void SwitchTo(int index);

        /**
         * @brief 現在の武器を取得
         * @return 現在装備中の武器。なければnullptr
         */
        IWeaponComponent* GetCurrentWeapon() const;

        /**
         * @brief 現在の武器インデックスを取得
         * @return 武器インデックス（0始まり）
         */
        int GetCurrentIndex() const { return currentIndex_; }

        /**
         * @brief 登録されている武器数を取得
         * @return 武器の総数
         */
        size_t GetWeaponCount() const { return weapons_.size(); }

    private:
        // 切り替え入力をチェック
        void ProcessSwitchInput();

        // 武器リスト（このコンポーネントが所有）
        std::vector<std::unique_ptr<IWeaponComponent>> weapons_;
        // 現在選択中の武器インデックス
        int currentIndex_ = 0;
        // 3Dオブジェクト共通情報（武器作成用）
        Object3dCommon* object3dCommon_ = nullptr;
        // ライトマネージャー（武器作成用）
        LightManager* lightManager_ = nullptr;
    };
}
